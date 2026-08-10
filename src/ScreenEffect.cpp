#include "th_pch.h"

#include "ScreenEffect.hpp"

#include "AnmManager.hpp"
#include "GameManager.hpp"

namespace th08
{

DIFFABLE_STATIC(i32, g_ScreenEffectCounter);
DIFFABLE_STATIC(ScreenEffect, g_ScreenEffect);

ScreenEffect::ScreenEffect()
{
}

void ScreenEffect::Clear(D3DCOLOR color)
{
    // ZUN bloat: This is doing the exact same thing twice
    g_Supervisor.d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0);
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
    g_Supervisor.d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 1.0f, 0);
    if (FAILED(g_Supervisor.d3dDevice->Present(NULL, NULL, NULL, NULL)))
    {
        g_Supervisor.d3dDevice->Reset(&g_Supervisor.presentParameters);
    }
}

void ScreenEffect::SetViewport(D3DCOLOR clearColor)
{
    if (g_AnmManager)
    {
        g_AnmManager->FlushVertexBuffer();
    }
    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = 640;
    g_Supervisor.viewport.Height = 480;
    g_Supervisor.viewport.MinZ = 0.0f;
    g_Supervisor.viewport.MaxZ = 1.0f;
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    Clear(clearColor);
}

ChainCallbackResult ScreenEffect::CalcFadeIn(ScreenEffect *screenEffect)
{
    if (screenEffect->duration != 0)
    {
        screenEffect->alpha =
            (i32)(255.0f - (f32)screenEffect->timer * 255.0f / (f32)screenEffect->duration);
        if (screenEffect->alpha < 0)
        {
            screenEffect->alpha = (i32)0.0f;
        }
    }
    if (screenEffect->timer >= screenEffect->duration)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }

    screenEffect->timer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

void ScreenEffect::DrawSquare(ZunRect *rectDimensions, D3DCOLOR color)
{
    g_AnmManager->FlushVertexBuffer();

    VertexDiffuseXyzrhw vertices[4];

    vertices[0].pos = Float3(rectDimensions->left, rectDimensions->top, 0.0f);
    vertices[1].pos = Float3(rectDimensions->right, rectDimensions->top, 0.0f);
    vertices[2].pos = Float3(rectDimensions->left, rectDimensions->bottom, 0.0f);
    vertices[3].pos = Float3(rectDimensions->right, rectDimensions->bottom, 0.0f);
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = 1.0f;
    vertices[0].diffuse = vertices[1].diffuse = vertices[2].diffuse = vertices[3].diffuse = color;
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 0);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 0);
    if (!g_Supervisor.IsDepthTestDisabled())
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, 6);
    g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_DIFFUSE | D3DFVF_XYZRHW);
    g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(VertexDiffuseXyzrhw));
    g_AnmManager->ClearVertexShader();
    g_AnmManager->ClearSprite();
    g_AnmManager->ClearTexture();
    g_AnmManager->ClearColorOp();
    g_AnmManager->ClearBlendMode();
    g_AnmManager->ClearZWriteSetting();
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
}

void ScreenEffect::DrawSquareShaded(ZunRect *rect, D3DCOLOR topLeft, D3DCOLOR topRight, D3DCOLOR bottomLeft,
                                    D3DCOLOR bottomRight)
{
    g_AnmManager->FlushVertexBuffer();

    VertexDiffuseXyzrhw vertices[4];

    vertices[0].pos = Float3(rect->left, rect->top, 0.0f);
    vertices[1].pos = Float3(rect->right, rect->top, 0.0f);
    vertices[2].pos = Float3(rect->left, rect->bottom, 0.0f);
    vertices[3].pos = Float3(rect->right, rect->bottom, 0.0f);
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w = 1.0f;
    vertices[0].diffuse = topLeft;
    vertices[1].diffuse = topRight;
    vertices[2].diffuse = bottomLeft;
    vertices[3].diffuse = bottomRight;
    if (!g_Supervisor.IsColorCompositingDisabled())
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 2);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 2);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 0);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 0);
    if (!g_Supervisor.IsDepthTestDisabled())
    {
        g_Supervisor.d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    }
    g_Supervisor.d3dDevice->SetRenderState(D3DRS_DESTBLEND, 6);
    g_Supervisor.d3dDevice->SetVertexShader(D3DFVF_DIFFUSE | D3DFVF_XYZRHW);
    g_Supervisor.d3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(VertexDiffuseXyzrhw));
    g_AnmManager->ClearVertexShader();
    g_AnmManager->ClearSprite();
    g_AnmManager->ClearTexture();
    g_AnmManager->ClearColorOp();
    g_AnmManager->ClearBlendMode();
    g_AnmManager->ClearZWriteSetting();
    if (!g_Supervisor.IsColorCompositingDisabled())
    {
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, 4);
        g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, 4);
    }
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, 2);
    g_Supervisor.d3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, 2);
}

ChainCallbackResult ScreenEffect::CalcFadeOut(ScreenEffect *screenEffect)
{
    if (g_ScreenEffectCounter != 0)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }
    if (screenEffect->duration != 0)
    {
        screenEffect->alpha = (i32)((f32)screenEffect->timer * 255.0f / (f32)screenEffect->duration);
        if (screenEffect->alpha < 0)
        {
            screenEffect->alpha = (i32)0.0f;
        }
    }
    if (screenEffect->timer >= screenEffect->duration)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }
    if (g_GameManager.isInGameMenu == 0 && g_GameManager.showRetryMenu == 0)
    {
        screenEffect->timer++;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ScreenEffect::CalcPartialFadeOut(ScreenEffect *screenEffect)
{
    if (screenEffect->unk24 == 0)
    {
        if (screenEffect->duration != 0)
        {
            if (screenEffect->timer <= screenEffect->duration)
            {
                screenEffect->alpha =
                    (i32)((f32)screenEffect->timer * 128.0f / (f32)screenEffect->duration);
            }
        }
    }
    else if (screenEffect->timer <= 8)
    {
        screenEffect->alpha = 0x80 - (i32)((f32)screenEffect->timer * 128.0f / 8.0f);
    }
    else
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }
    screenEffect->timer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ScreenEffect::CalcPulse(ScreenEffect *screenEffect)
{
    u32 maxAlpha = (screenEffect->args[1] >> 24) & 0xff;
    if (g_ScreenEffectCounter != 0)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }
    if (screenEffect->timer < screenEffect->duration)
    {
        screenEffect->alpha = maxAlpha - (i32)((f32)screenEffect->timer * maxAlpha / screenEffect->duration);
        if (screenEffect->alpha < 0)
        {
            screenEffect->alpha = 0;
        }
    }
    else
    {
        screenEffect->alpha = 0;
        screenEffect->args[0]--;
        if ((i32)screenEffect->args[0] <= 0)
        {
            return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
        }
        screenEffect->timer = 0;
    }
    screenEffect->timer++;
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

#pragma var_order(calcElem, drawElem, screenEffect)
ScreenEffect *ScreenEffect::RegisterChain(ScreenEffectType effect, i32 ticks, i32 param_3, i32 param_4, i32 param_5,
                                          i32 param_6)
{
    ChainElem *calcElem = NULL;
    ChainElem *drawElem = NULL;

    ScreenEffect *screenEffect = ZUN_NEW(ScreenEffect, "ScreenInf");
    if (screenEffect == NULL)
    {
        return NULL;
    }
    memset(screenEffect, 0, sizeof(ScreenEffect));
    switch (effect)
    {
    case SCREEN_EFFECT_FULL_FADE_IN:
        calcElem = g_Chain.CreateElem((ChainCallback)CalcFadeIn);
        drawElem = g_Chain.CreateElem((ChainCallback)DrawFullFade);
        break;
    case SCREEN_EFFECT_SHAKE:
        calcElem = g_Chain.CreateElem((ChainCallback)CalcShake);
        break;
    case SCREEN_EFFECT_ARCADE_FADE_OUT:
        calcElem = g_Chain.CreateElem((ChainCallback)CalcFadeOut);
        drawElem = g_Chain.CreateElem((ChainCallback)DrawArcadeFade);
        break;
    case SCREEN_EFFECT_FULL_FADE_OUT:
        calcElem = g_Chain.CreateElem((ChainCallback)CalcFadeOut);
        drawElem = g_Chain.CreateElem((ChainCallback)DrawFullFade);
        break;
    case SCREEN_EFFECT_UNK3:
        calcElem = g_Chain.CreateElem((ChainCallback)CalcPulse);
        drawElem = g_Chain.CreateElem((ChainCallback)DrawPlayAreaPulse);
        break;
    case SCREEN_EFFECT_UNK5:
        calcElem = g_Chain.CreateElem((ChainCallback)CalcPartialFadeOut);
        drawElem = g_Chain.CreateElem((ChainCallback)DrawPartialFade);
        break;
    case SCREEN_EFFECT_UNK6:
        calcElem = g_Chain.CreateElem((ChainCallback)CalcPartialFadeOut);
        drawElem = g_Chain.CreateElem((ChainCallback)DrawArcadeFade);
        break;
    case SCREEN_EFFECT_UNK7:
        calcElem = g_Chain.CreateElem((ChainCallback)CalcShakeWithEnvelope);
    }
    calcElem->addedCallback = (ChainLifetimeCallback)AddedCallback;
    calcElem->deletedCallback = (ChainLifetimeCallback)DeletedCallback;
    calcElem->arg = screenEffect;
    screenEffect->effect = effect;
    screenEffect->duration = ticks;
    screenEffect->args[0] = param_3;
    screenEffect->args[1] = param_4;
    screenEffect->args[2] = param_5;
    if (g_Chain.AddToCalcChain(calcElem, 3))
    {
        return NULL;
    }

    if (drawElem)
    {
        drawElem->arg = screenEffect;
        g_Chain.AddToDrawChain(drawElem, param_6);
    }
    screenEffect->calcChain = calcElem;
    screenEffect->drawChain = drawElem;
    return screenEffect;
}

ChainCallbackResult ScreenEffect::DrawFullFade(ScreenEffect *screenEffect)
{
    ZunRect rect;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = 640.0f;
    rect.bottom = 480.0f;
    g_AnmManager->FlushVertexBuffer();
    g_Supervisor.viewport.X = 0;
    g_Supervisor.viewport.Y = 0;
    g_Supervisor.viewport.Width = 640;
    g_Supervisor.viewport.Height = 480;
    g_Supervisor.d3dDevice->SetViewport(&g_Supervisor.viewport);
    ScreenEffect::DrawSquare(&rect, screenEffect->alpha << 24 | screenEffect->args[0]);
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ScreenEffect::DrawPartialFade(ScreenEffect *screenEffect)
{
    ZunRect rect;

    rect.left = 0.0f;
    rect.top = 0.0f;
    rect.right = 640.0f;
    rect.bottom = 480.0f;
    ScreenEffect::DrawSquare(&rect, screenEffect->alpha << 24 | screenEffect->args[0]);
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ScreenEffect::DrawPlayAreaPulse(ScreenEffect *screenEffect)
{
    ZunRect rect;

    rect.left = 32.0f;
    rect.top = 16.0f;
    rect.right = 416.0f;
    rect.bottom = 464.0f;
    ScreenEffect::DrawSquare(&rect, screenEffect->alpha << 24 | (screenEffect->args[1] & 0xffffff));
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ScreenEffect::DrawArcadeFade(ScreenEffect *screenEffect)
{
    ZunRect rect;

    rect.left = 32.0f;
    rect.top = 16.0f;
    rect.right = 416.0f;
    rect.bottom = 464.0f;
    ScreenEffect::DrawSquare(&rect, screenEffect->alpha << 24 | screenEffect->args[0]);
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ScreenEffect::CalcShake(ScreenEffect *screenEffect)
{
    if (g_GameManager.flags.unk10)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if (g_GameManager.unk2C != 0)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if (g_ScreenEffectCounter != 0)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }

    screenEffect->timer++;
    if (screenEffect->timer >= screenEffect->duration)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }

    f32 fVar1 = (f32)screenEffect->timer * (i32)(screenEffect->args[1] - screenEffect->args[0]);
    fVar1 /= (f32)screenEffect->duration;
    fVar1 += (f32)(i32)screenEffect->args[0];
    switch (g_Rng.GetRandomU32InRange(3))
    {
    case 0:
        g_AnmManager->screenShakeOffset.x = 0.0f;
        break;
    case 1:
        g_AnmManager->screenShakeOffset.x = fVar1;
        break;
    case 2:
        g_AnmManager->screenShakeOffset.x = -fVar1;
    }
    switch (g_Rng.GetRandomU32InRange(3))
    {
    case 0:
        g_AnmManager->screenShakeOffset.y = 0.0f;
        break;
    case 1:
        g_AnmManager->screenShakeOffset.y = fVar1;
        break;
    case 2:
        g_AnmManager->screenShakeOffset.y = -fVar1;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

ChainCallbackResult ScreenEffect::CalcShakeWithEnvelope(ScreenEffect *screenEffect)
{
    if (g_GameManager.flags.unk10)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if (g_GameManager.unk2C != 0)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE;
    }

    if ((i32)g_GameManager.unk3ddc0 <= 1)
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }

    screenEffect->timer++;

    f32 amount;
    if (screenEffect->timer < (i32)screenEffect->args[0])
    {
        amount = (f32)screenEffect->timer / (i32)screenEffect->args[0];
    }
    else if (screenEffect->timer < (i32)(screenEffect->args[0] + screenEffect->args[1]))
    {
        amount = 1.0f;
    }
    else if (screenEffect->timer < (i32)(screenEffect->args[0] + screenEffect->args[1] + screenEffect->args[2]))
    {
        amount = ((f32)(screenEffect->args[0] + screenEffect->args[1] + screenEffect->args[2]) -
                  (f32)screenEffect->timer) /
                 (f32)screenEffect->args[2];
    }
    else
    {
        return CHAIN_CALLBACK_RESULT_CONTINUE_AND_REMOVE_JOB;
    }
    amount = amount * screenEffect->duration;
    switch (g_Rng.GetRandomU32InRange(3))
    {
    case 0:
        g_AnmManager->screenShakeOffset.x = 0.0f;
        break;
    case 1:
        g_AnmManager->screenShakeOffset.x = amount;
        break;
    case 2:
        g_AnmManager->screenShakeOffset.x = -amount;
    }
    switch (g_Rng.GetRandomU32InRange(3))
    {
    case 0:
        g_AnmManager->screenShakeOffset.y = 0.0f;
        break;
    case 1:
        g_AnmManager->screenShakeOffset.y = amount;
        break;
    case 2:
        g_AnmManager->screenShakeOffset.y = -amount;
    }
    return CHAIN_CALLBACK_RESULT_CONTINUE;
}

void ScreenEffect::Stop()
{
    this->unk24 = 1;
    this->timer = 0;
}

ZunResult ScreenEffect::AddedCallback(ScreenEffect *screenEffect)
{
    screenEffect->timer = 0;
    return ZUN_SUCCESS;
}

ZunResult ScreenEffect::DeletedCallback(ScreenEffect *screenEffect)
{
    screenEffect->calcChain->deletedCallback = NULL;
    g_Chain.Cut(screenEffect->drawChain);
    screenEffect->drawChain = NULL;
    ZUN_DELETE(screenEffect);
    return ZUN_SUCCESS;
}

} /* namespace th08 */
