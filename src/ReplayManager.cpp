#include "th_pch.h"

#include "Global.hpp"
#include "GameManager.hpp"
#include "Player.hpp"
#include "ReplayManager.hpp"
#include "ResultScreen.hpp"

#include "pbg/Lzss.hpp"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace th08
{

DIFFABLE_STATIC(ReplayManager *, g_ReplayManager);

// Matches the declaration in EclManager.cpp so the call below resolves to the
// same out-of-line symbol (EclSpellcardVars::GetTimeOrbsExtra @ 0x453cc0).
class EclSpellcardVars
{
  public:
    i32 GetTimeOrbsExtra();
};

void ReplayManager::StopRecording()
{
    ReplayManager *replayManager = g_ReplayManager;

    if (replayManager != NULL)
    {
        replayManager->replayEventCursor += 2;
        *(u16 *)replayManager->replayEventCursor = 0;
        replayManager->stageReplayDataStart[g_GameManager.currentStage] = replayManager->replayEventCursor + 6;
    }
}

// FUNCTION: th08 0x4531f0
#pragma var_order(i, rm, cursor, bytesWritten, encoded, slowRate, encodedSize, stageSize, pBuf, slowRatePct, localCopy, textBuffer, fileHandle, textBlockSize, userMagic, dataSize, timeinfo, t, timeBuf, obfuscate, ptr, obfuscateOffset, ptr2)
void ReplayManager::SaveReplay(const char *replayPath, const char *replayName)
{
    i32 i;
    ReplayManager *rm;
    char *cursor;
    DWORD bytesWritten;
    u8 *encoded;
    f32 slowRate;
    i32 encodedSize;
    i32 stageSize;
    u8 *pBuf;
    f32 slowRatePct;
    ReplayData localCopy;
    char textBuffer[0x400];
    HANDLE fileHandle;
    i32 textBlockSize;
    u8 userMagic[0xc];
    u32 dataSize;
    struct tm *timeinfo;
    time_t t;
    char timeBuf[0x100];
    i32 obfuscate;
    u8 *ptr;
    u8 obfuscateOffset;
    u8 *ptr2;

    if (g_ReplayManager == NULL)
    {
        goto ret;
    }

    rm = g_ReplayManager;

    if (((EclSpellcardVars *)rm)->GetTimeOrbsExtra() != 0)
    {
        goto chainCut;
    }

    if (!g_GameManager.IsPracticeMode())
    {
        if (g_GameManager.difficulty < 4 &&
            memcmp((const void *)0x17ce870, &rm->replayData->gameConfiguration, sizeof(GameConfiguration)) != 0)
        {
            goto freeStages;
        }
    }

    if (rm->replayData->gameConfiguration.slowMode != 0)
    {
        goto freeStages;
    }

    if (replayPath == NULL)
    {
        goto freeStages;
    }

    utils::DebugPrint("info : Replay File write %s\r\n", replayPath);

    pBuf = (u8 *)g_ZunMemory.Alloc(0x400000, "rep tmp");
    memcpy(&localCopy, rm->replayData, sizeof(ReplayData));

    ReplayManager::StopRecording();

    i = g_GameManager.currentStage2;
    rm->replayData->header.stageReplayData[i]->score = g_GameManager.globals->score;

    dataSize = sizeof(ReplayDataHeader);
    dataSize += 0xcc;

    for (i = 0; i < MAX_STAGES; i++)
    {
        if (rm->replayData->header.stageReplayData[i] != NULL)
        {
            stageSize = (i32)(rm->stageReplayDataStart[i] - (u8 *)rm->replayData->header.stageReplayData[i]);
            memcpy(pBuf + dataSize - sizeof(ReplayDataHeader), rm->replayData->header.stageReplayData[i], stageSize);
            localCopy.header.stageReplayData[i] = (StageReplayData *)(dataSize - sizeof(ReplayDataHeader));
            dataSize += stageSize;
        }
    }

    for (i = 0; i < MAX_STAGES; i++)
    {
        if (rm->replayData->header.stageReplayData2[i] != NULL)
        {
            stageSize = (i32)(((u8 **)((u8 *)rm + 0xa4))[i] - (u8 *)rm->replayData->header.stageReplayData2[i]);
            memcpy(pBuf + dataSize - sizeof(ReplayDataHeader), rm->replayData->header.stageReplayData2[i], stageSize);
            localCopy.header.stageReplayData2[i] = (StageReplayData *)(dataSize - sizeof(ReplayDataHeader));
            dataSize += stageSize;
        }
    }

    localCopy.spellcardScore = g_GameManager.globals->displayScore;

    if (localCopy.spellcardNumber >= 0)
    {
        memcpy(localCopy.spellcardName, g_GameManager.catkData[localCopy.spellcardNumber].spellName,
               sizeof(localCopy.spellcardName));
    }

    slowRate = (*(f32 *)0x17ce8e8 / *(f32 *)0x17ce8ec - 0.5f) * 2.0f;
    if (slowRate < 0.0f)
    {
        slowRate = 0.0f;
    }
    if (slowRate >= 1.0f)
    {
        slowRate = 1.0f;
    }
    slowRatePct = (1.0f - slowRate) * 100.0f;

    *(u32 *)userMagic = *(u32 *)0x4b7190;
    userMagic[8] = 0;

    memset(textBuffer, 0, sizeof(textBuffer));
    cursor = textBuffer;

    cursor = (char *)sprintf(cursor, "\203\166\203\214\203\103\203\204\201\133\226\274\011\045\163\015\012", replayName);

    time(&t);
    timeinfo = localtime(&t);
    strftime(timeBuf, 0x14, "%Y/%m/%d %H:%M:%S", timeinfo);
    cursor = (char *)sprintf(cursor, "\203\166\203\214\203\103\216\236\215\217\011\045\163\015\012", timeBuf);

    cursor = (char *)sprintf(cursor, "\203\114\203\203\203\211\226\274\011\045\163\015\012", ResultScreen::GetCharacterName(g_PlayerCharacter));
    cursor = (char *)sprintf(cursor, "\203\130\203\122\203\101\011\011\045\144\060\015\012", (i32)g_GameManager.globals->displayScore);
    cursor = (char *)sprintf(cursor, "\223\357\210\325\223\170\011\011\045\163\015\012", ((const char **)0x4c7f30)[g_GameManager.difficulty]);

    if (localCopy.spellcardNumber >= 0)
    {
        cursor = (char *)sprintf(cursor, "\203\112\201\133\203\150\226\274\011\116\157\056\045\063\144\040\045\163\015\012", localCopy.spellcardNumber + 1, localCopy.spellcardName);
    }
    else
    {
        const char *stageName;
        if ((g_PlayerFlags >> 4) & 1)
        {
            stageName = "Clear";
        }
        else
        {
            stageName = ResultScreen::GetStageName(g_Unknown164d2cc);
        }
        cursor = (char *)sprintf(cursor, "\215\305\217\111\203\130\203\145\201\133\203\127\011\045\163\015\012", stageName);
    }

    cursor = (char *)sprintf(cursor, "\203\176\203\130\211\361\220\224\011\045\144\015\012", g_GameManager.GetDeaths());
    cursor = (char *)sprintf(cursor, "\203\173\203\200\211\361\220\224\011\045\144\015\012", g_GameManager.GetBombsUsed());
    cursor = (char *)sprintf(cursor, "\217\210\227\235\227\216\202\277\227\246\011\045\146\045\045\015\012", (double)slowRatePct);

    *(i32 *)0x164cfb8 = (i32)((f32)*(i32 *)0x164d0a8 / *(i32 *)0x164d0ac * 10000.0f);
    cursor = (char *)sprintf(cursor, "\220\154\212\324\227\246\011\011\045\063\056\062\146\201\223\015\012", (double)((f32)*(i32 *)0x164cfb8 / 100.0f));

    cursor = (char *)sprintf(cursor, "\203\121\201\133\203\200\202\314\203\157\201\133\203\127\203\207\203\223\011\045\144\056\045\056\062\144\045\143\015\012", 1, 0, 'd');

    textBlockSize = (i32)strlen(textBuffer) + 0xc;
    textBlockSize += textBlockSize & 1;

    localCopy.header.unk0x7 = 1;
    strcpy(localCopy.playerName, replayName);
    ResultScreen::FormatDate(localCopy.date);

    localCopy.header.value1 = (u8)(g_Rng.GetRandomU16InRange(0x80) + 0x40);
    localCopy.unk0x68 = (u8)g_Rng.GetRandomU16InRange(0x100);
    localCopy.header.unk0x14 = (u8)g_Rng.GetRandomU16InRange(0x100);

    memcpy(pBuf, &localCopy.unk0x68, 0xcc);

    utils::DebugPrint("info : original size %d\r\n", dataSize);

    localCopy.header.decompressedSize = (i32)dataSize - (i32)sizeof(ReplayDataHeader);
    encoded = Lzss::Encode(pBuf, localCopy.header.decompressedSize, &localCopy.header.compressedSize);
    encodedSize = localCopy.header.compressedSize;
    g_ZunMemory.Free(pBuf);

    obfuscate = REPLAY_OBFUSCATION_VALUE;
    ptr = &localCopy.header.value1;
    for (i = 0; (u32)i < 0x53; i++, ptr++)
    {
        obfuscate += *ptr;
    }
    ptr = encoded;
    for (i = 0; i < encodedSize; i++, ptr++)
    {
        obfuscate += *ptr;
    }
    localCopy.header.checksum = obfuscate;

    ptr2 = (u8 *)&localCopy.header.compressedSize;
    obfuscateOffset = localCopy.header.value1;
    for (i = 0; (u32)i < 0x50; i++, ptr2++)
    {
        *ptr2 += obfuscateOffset;
        obfuscateOffset += 7;
    }
    ptr2 = encoded;
    for (i = 0; i < encodedSize; i++, ptr2++)
    {
        *ptr2 += obfuscateOffset;
        obfuscateOffset += 7;
    }

    fileHandle = CreateFileA(replayPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        goto freeStages;
    }

    WriteFile(fileHandle, &localCopy.header, sizeof(ReplayDataHeader), &bytesWritten, NULL);
    WriteFile(fileHandle, encoded, encodedSize, &bytesWritten, NULL);
    WriteFile(fileHandle, userMagic, 0xc, &bytesWritten, NULL);
    WriteFile(fileHandle, textBuffer, textBlockSize - 0xc, &bytesWritten, NULL);
    CloseHandle(fileHandle);

    utils::DebugPrint("info : Size %d -> %d\r\n", dataSize, encodedSize + 0x68);
    GlobalFree(encoded);

freeStages:
    for (i = 0; i < MAX_STAGES; i++)
    {
        if (g_ReplayManager->replayData->header.stageReplayData[i] != NULL)
        {
            g_ZunMemory.Free(g_ReplayManager->replayData->header.stageReplayData[i]);
        }
        if (g_ReplayManager->replayData->header.stageReplayData2[i] != NULL)
        {
            g_ZunMemory.Free(g_ReplayManager->replayData->header.stageReplayData2[i]);
        }
    }

chainCut:
    g_Chain.Cut(g_ReplayManager->replayChainElem);

ret:
    ;
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

// FUNCTION: th08 0x453160
ReplayManager::ReplayManager()
{
}

// STUB: th08 0x4522a0
// Per-frame replay update (also registered as a calc-chain callback).
// Clears replayEventFlags, snapshots the replay RNG value into replayRngValue,
// resets the replay RNG call counter, and promotes the "quit requested" global
// flag into replayEventFlags bit 0x100 before clearing it.
void ReplayManager::UpdateReplay()
{
}

// STUB: th08 0x452830
// Starts replay recording: allocates a fresh ReplayData, fills the header from
// the current game state, and sets up per-stage replay event buffers.
void ReplayManager::StartRecording()
{
}

// STUB: th08 0x452d60
// Starts replay playback: loads + LZSS-decodes the replay file at replayPath
// into replayData (re-allocating per-stage buffers for stage replay).
void ReplayManager::StartReplay()
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
            rm->replayData = NULL;
            rm->replayMode = param_1;
            rm->replayPath = path;
            switch (param_1)
            {
            case 0:
                elem = g_Chain.CreateElem((ChainCallback)0x452310);
                rm->replayChainElem = elem;
                elem->addedCallback = (ChainLifetimeCallback)0x452830;
                elem->deletedCallback = (ChainLifetimeCallback)0x453080;
                elem->arg = rm;
                if (g_Chain.AddToCalcChain(elem, 0x11) != 0)
                {
                    return (ZunResult)-1;
                }
                rm->keyInputChainElem = NULL;
                elem = g_Chain.CreateElem((ChainCallback)0x4522a0);
                rm->replayUpdateChainElem = elem;
                elem->arg = rm;
                g_Chain.AddToCalcChain(elem, 7);
                rm->UpdateReplay();
                break;
            case 1:
                elem = g_Chain.CreateElem((ChainCallback)0x452550);
                rm->replayChainElem = elem;
                elem->addedCallback = (ChainLifetimeCallback)0x452d60;
                elem->deletedCallback = (ChainLifetimeCallback)0x453080;
                elem->arg = rm;
                if (g_Chain.AddToCalcChain(elem, 6) != 0)
                {
                    return (ZunResult)-1;
                }
                if (((ReplayDataHeader *)rm->replayData)->unk0x6 != 0)
                {
                    elem->callback = (ChainCallback)0x4526c0;
                }
                elem = g_Chain.CreateElem((ChainCallback)0x452490);
                rm->keyInputChainElem = elem;
                elem->arg = rm;
                g_Chain.AddToCalcChain(elem, 0x12);
                rm->replayUpdateChainElem = NULL;
                if (((ReplayDataHeader *)rm->replayData)->unk0x6 != 0)
                {
                    elem = g_Chain.CreateElem((ChainCallback)0x4522a0);
                    rm->replayUpdateChainElem = elem;
                    elem->arg = rm;
                    g_Chain.AddToCalcChain(elem, 7);
                    rm->UpdateReplay();
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
            g_ReplayManager->StartRecording();
            break;
        case 1:
            g_ReplayManager->StartReplay();
            break;
        }
    }
    return ZUN_SUCCESS;
}

} // namespace th08
