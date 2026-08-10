#include "th_pch.h"

#include "Background.hpp"
#include "AnmManager.hpp"

namespace th08
{
DIFFABLE_STATIC(Background, g_Background);
DIFFABLE_STATIC(ChainElem, g_BackgroundCalcChain);
DIFFABLE_STATIC(ChainElem, g_BackgroundDrawChainHighPrio);
DIFFABLE_STATIC(ChainElem, g_BackgroundDrawChainLowPrio);

DIFFABLE_STATIC_ARRAY_ASSIGN(const char *, 9, g_StageAnmFiles) = {
    "stg1bg.anm", "stg2bg.anm", "stg3bg.anm", "stg4abg.anm", "stg4abg.anm",
    "stg5bg.anm", "stg6bg.anm", "stg7bg.anm", "stg8bg.anm",
};
DIFFABLE_STATIC_ARRAY_ASSIGN(const char *, 9, g_StageStdFiles) = {
    "stage1.std", "stage2.std", "stage3.std", "stage4a.std", "stage4b.std",
    "stage5.std", "stage6.std", "stage7.std", "stage8.std",
};
DIFFABLE_STATIC_ARRAY_ASSIGN(const char *, 9, g_StageStdFilesSpellPractice) = {
    "stage1_s.std", "stage2_s.std", "stage3_s.std", "stage4a_s.std", "stage4b_s.std",
    "stage5_s.std", "stage6_s.std", "stage7_s.std", "stage8_s.std",
};

Background::Background()
{
    memset(this, 0, sizeof(Background));
    this->camera4.pos = Float3(0.0f, 0.0f, 1000.0f);
    this->camera4.target = Float3(0.0f, 0.0f, 0.0f);
    this->camera4.up = Float3(0.0f, 1.0f, 0.0f);
    this->camera4.fov = ZUN_PI / 6.0f;
    this->camera0 = this->camera4;
    this->camera1 = this->camera4;
}

// STUB: th08 0x407400
ChainCallbackResult Background::OnUpdate(Background *background)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x409200
ChainCallbackResult Background::OnDrawHighPrio(Background *background)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// STUB: th08 0x409640
ChainCallbackResult Background::OnDrawLowPrio(Background *background)
{
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

// FUNCTION: th08 0x409850
ZunResult Background::AddedCallback(Background *background)
{
    background->timer0x80c = 0;
    *(u32 *)&background->unk0x818 = 0;
    background->unk0x824.x = 0.0f;
    background->unk0x824.y = 0.0f;
    background->unk0x824.z = 0.0f;
    background->unk0xb24 = 0;
    background->unk0xb10 = 0;

    if (!IsDisableResourceReload())
    {
        background->stageAnm = g_AnmManager->PreloadAnm(4, g_StageAnmFiles[g_GameManager.currentStage]);
        if (background->stageAnm == NULL)
        {
            return ZUN_ERROR;
        }
    }
    else
    {
        background->stageAnm = g_AnmManager->GetAnm(4);
    }

    if (!g_GameManager.IsSpellPractice())
    {
        if (background->LoadStageData((char *)g_StageStdFiles[g_GameManager.currentStage]))
        {
            return ZUN_ERROR;
        }
    }
    else
    {
        if (background->LoadStageData((char *)g_StageStdFilesSpellPractice[g_GameManager.currentStage]))
        {
            return ZUN_ERROR;
        }
    }

    background->fog.color.d3dColor = 0xff000000;
    background->fog.nearPlane = 200.0f;
    background->fog.farPlane = 500.0f;

    background->camera4.pos = Float3(0.0f, 0.0f, 1000.0f);
    background->camera4.target = Float3(0.0f, 0.0f, 0.0f);
    background->camera4.unk0x3c = Float3(0.0f, 0.0f, 0.0f);
    background->camera4.up = Float3(0.0f, 1.0f, 0.0f);
    background->camera4.fov = ZUN_PI / 6.0f;

    background->camera0 = background->camera4;
    background->camera1 = background->camera4;

    background->unk0x6474 = 0;

    for (i32 i = 0; i < 4; i++)
    {
        background->unk0x63e0[i] = 0;
        background->timers0x63f4[i] = 0;
    }

    background->unk0x6260 = 0;
    *(u32 *)&background->unk0x6470 = 0x49a17020;

    if (g_GameManager.currentStage == 5)
    {
        *(u32 *)&background->unk0x6470 = 0x49de7920;
    }
    else if (g_GameManager.currentStage == 6 || g_GameManager.currentStage == 7)
    {
        *(u32 *)&background->unk0x6470 = 0x4a45c100;
    }

    return ZUN_SUCCESS;
}

ZunResult Background::RegisterChain(i32 stage)
{
    Background *background = &g_Background;
    StdRawHeader *savedStdData;

    if (IsDisableResourceReload())
    {
        savedStdData = background->stdData;
    }
    memset(background, 0, sizeof(Background));
    if (IsDisableResourceReload())
    {
        background->stdData = savedStdData;
    }
    background->unk81c = 0;
    background->currentStage = stage;
    g_BackgroundCalcChain.SetCallback((ChainCallback)OnUpdate);
    g_BackgroundCalcChain.addedCallback = (ChainLifetimeCallback)AddedCallback;
    g_BackgroundCalcChain.deletedCallback = (ChainLifetimeCallback)DeletedCallback;
    g_BackgroundCalcChain.arg = background;
    if (g_Chain.AddToCalcChain(&g_BackgroundCalcChain, 8))
    {
        return ZUN_ERROR;
    }
    g_BackgroundDrawChainHighPrio.SetCallback((ChainCallback)OnDrawHighPrio);
    g_BackgroundDrawChainHighPrio.arg = background;
    g_Chain.AddToDrawChain(&g_BackgroundDrawChainHighPrio, 6);
    g_BackgroundDrawChainLowPrio.SetCallback((ChainCallback)OnDrawLowPrio);
    g_BackgroundDrawChainLowPrio.arg = background;
    g_Chain.AddToDrawChain(&g_BackgroundDrawChainLowPrio, 7);
    return ZUN_SUCCESS;
}

ZunResult Background::DeletedCallback(Background *background)
{
    if (!IsDisableResourceReload())
    {
        g_AnmManager->ReleaseAnm(4);
    }
    if (background->fileData != NULL)
    {
        g_ZunMemory.RemoveFromRegistry(background->fileData);
        background->fileData = NULL;
    }
    if (!IsDisableResourceReload())
    {
        if (background->stdData != NULL)
        {
            g_ZunMemory.RemoveFromRegistry(background->stdData);
            background->stdData = NULL;
        }
    }
    return ZUN_SUCCESS;
}

void Background::CutChain()
{
    g_Chain.Cut(&g_BackgroundCalcChain);
    g_Chain.Cut(&g_BackgroundDrawChainHighPrio);
    g_Chain.Cut(&g_BackgroundDrawChainLowPrio);
}

#pragma var_order(vmIdx, i, obj, quad)
ZunResult Background::LoadStageData(char *stdPath)
{
    i32 vmIdx;
    i32 i;
    StdRawObject *obj;
    StdRawQuadBasic *quad;

    if (IsDisableResourceReload() == 0)
    {
        this->stdData = (StdRawHeader *)FileSystem::OpenFile(stdPath, NULL, FALSE);
        if (this->stdData == NULL)
        {
            g_GameErrorContext.Log("\x83X\x83" "e\x81[\x83W\x83" "f\x81[\x83^\x82\xaa\x8c\xa9\x82\xc2\x82\xa9\x82\xe8\x82\xdc\x82\xb9\x82\xf1\x81" "B\x83" "f\x81[\x83^\x82\xaa\x89\xf3\x82\xea\x82\xc4\x82\xa2\x82\xdc\x82\xb7\x0d\x0a");
            return ZUN_ERROR;
        }
    }
    this->objectsCount = this->stdData->objectsCount;
    this->quadCount = this->stdData->quadCount;
    this->objectInstances = (StdRawInstance *)(this->stdData->facesOffset + (i32)this->stdData);
    this->beginningOfScript = (StdRawInstr *)(this->stdData->scriptOffset + (i32)this->stdData);
    this->objects = (StdRawObject **)(this->stdData + 1);
    if (IsDisableResourceReload() == 0)
    {
        for (i = 0; i < this->objectsCount; i++)
        {
            this->objects[i] = (StdRawObject *)((i32)this->objects[i] + (i32)this->stdData);
        }
    }
    this->fileData = g_ZunMemory.Alloc(this->quadCount * sizeof(AnmVm), "bgscroll");
    for (i = 0, vmIdx = 0; i < this->objectsCount; i++)
    {
        obj = this->objects[i];
        obj->flags = 1;
        quad = &obj->firstQuad;
        while (quad->type >= 0)
        {
            this->stageAnm->ExecuteAnmIdx(&((AnmVm *)this->fileData)[vmIdx], quad->anmScript);
            quad->vmIndex = vmIdx++;
            quad = (StdRawQuadBasic *)((i32)quad + quad->byteSize);
        }
    }
    switch (g_GameManager.currentStage)
    {
    case 2:
        g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->unk0x844, 0x21);
        break;
    default:
        g_Supervisor.textAnm->SetAndExecuteScriptIdx(&this->unk0x844, 0x21);
        break;
    }
    this->unk0x844.SetInterrupt(2);
    this->unk0x834 = 0;
    this->timer0x838 = 0;
    return ZUN_SUCCESS;
}

}; // Namespace th08
