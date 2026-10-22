#ifndef SHADER_SPOT_HLSLI
#define SHADER_SPOT_HLSLI

#include "Shader_Header.hlsli"
#include "Shader_ShadowCommon.hlsli"
#include "Shader_ShadowData.hlsli"
#include "Shader_RenderTuning.hlsli"

#define MAX_SHADOW_SPOTS 4

Texture2D g_ShadowMap : register(t32);


float Compute_Shadow_Spot(float3 vWorldPos, int idx, float2 vPixel)
{
    if (idx < 0 || idx >= g_SpotInfo.y)
        return 1.f;
    ShadowView v = g_ShadowViews[g_SpotInfo.x + idx];

    float4 vClip = mul(float4(vWorldPos, 1.f), v.ViewProj);
    if (vClip.w <= 0.f)
        return 1.f;
    float3 vNDC = vClip.xyz / vClip.w;
    float2 vTileUV = vNDC.xy * float2(0.5f, -0.5f) + 0.5f;
    if (any(vTileUV < 0.f) || any(vTileUV > 1.f) || vNDC.z > 1.f)
        return 1.f;

    float fReceiver = vNDC.z - g_RenderTuning[0].fSpotBias;
    return Shadow_PCF_Atlas(g_ShadowMap, g_PointSampler, vTileUV, v.AtlasRect, fReceiver,
                            (float) g_SpotInfo.z, g_RenderTuning[0].fSpotFilterTexels, vPixel);
}


float Compute_Shadow_Spot_Hard(float3 vWorldPos, int idx)
{
    if (idx < 0 || idx >= g_SpotInfo.y)
        return 1.f;
    ShadowView v = g_ShadowViews[g_SpotInfo.x + idx];

    float4 vClip = mul(float4(vWorldPos, 1.f), v.ViewProj);
    if (vClip.w <= 0.f)
        return 1.f;
    float3 vNDC = vClip.xyz / vClip.w;
    float2 vTileUV = vNDC.xy * float2(0.5f, -0.5f) + 0.5f;
    if (any(vTileUV < 0.f) || any(vTileUV > 1.f) || vNDC.z > 1.f)
        return 1.f;

    float2 vUV = v.AtlasRect.xy + vTileUV * v.AtlasRect.zw;
    return (vNDC.z - g_RenderTuning[0].fSpotBias <= g_ShadowMap.Sample(g_PointSampler, vUV).r) ? 1.f : 0.f;
}

#endif