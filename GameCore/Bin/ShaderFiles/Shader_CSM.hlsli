#ifndef SHADER_CSM_HLSLI
#define SHADER_CSM_HLSLI

#include "Shader_Header.hlsli"
#include "Shader_ShadowCommon.hlsli"
#include "Shader_ShadowData.hlsli"
#include "Shader_RenderTuning.hlsli"

#define CSM_CASCADE_COUNT 4

Texture2D g_CSMMap : register(t33);

float Compute_Shadow_CSM(float3 vWorldPos, float fViewZ, float2 vPixel)
{
    if (g_CSMInfo.w == 0 || g_CSMInfo.y == 0)
        return 1.f;
    float s[CSM_CASCADE_COUNT] = { g_CSMSplitViewZ.x, g_CSMSplitViewZ.y, g_CSMSplitViewZ.z, g_CSMSplitViewZ.w };
    int c = g_CSMInfo.y - 1;
        [unroll]
    for (int i = 0; i < g_CSMInfo.y; ++i)
    {
        if (fViewZ <= s[i])
        {
            c = i;
            break;
        }
    }

    ShadowView v = g_ShadowViews[g_CSMInfo.x + c];
    float4 vClip = mul(float4(vWorldPos, 1.f), v.ViewProj);
    if (vClip.w <= 0.f)
        return 1.f;
    float3 vNDC = vClip.xyz / vClip.w;
    float2 vTileUV = vNDC.xy * float2(0.5f, -0.5f) + 0.5f;
    if (any(vTileUV < 0.f) || any(vTileUV > 1.f) || vNDC.z > 1.f)
        return 1.f;

    float fBias = g_RenderTuning[0].fCSMBiasBase * (float) (1 << c);
    float fReceiver = vNDC.z - fBias;
    return Shadow_PCF_Atlas(g_CSMMap, g_PointSampler, vTileUV, v.AtlasRect, fReceiver,
                                (float) g_CSMInfo.z, g_RenderTuning[0].fCSMFilterTexels, vPixel);
}
#endif