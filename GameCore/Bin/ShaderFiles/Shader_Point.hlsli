#ifndef SHADER_POINT_HLSLI
#define SHADER_POINT_HLSLI

#include "Shader_Header.hlsli"
#include "Shader_ShadowCommon.hlsli"
#include "Shader_ShadowData.hlsli"
#include "Shader_RenderTuning.hlsli"

#define MAX_POINT_SHADOWS 2


Texture2D g_PointMap : register(t34);

float Compute_Shadow_Point(float3 vWorldPos, int idx, float2 vPixel)
{
    if (idx < 0 || idx >= g_PointInfo.y)
        return 1.f;
    float3 vL = g_PointLightPoseRange[idx].xyz;
    float fRng = g_PointLightPoseRange[idx].w;
    float3 vDir = vWorldPos - vL;
    float fDist = length(vDir);
    if (fDist >= fRng)
        return 1.f;

    float3 a = abs(vDir);
    int face;
    if (a.x >= a.y && a.x >= a.z)
        face = (vDir.x > 0) ? 0 : 1;
    else if (a.y >= a.z)
        face = (vDir.y > 0) ? 2 : 3;
    else
        face = (vDir.z > 0) ? 4 : 5;

    ShadowView v = g_ShadowViews[g_PointInfo.x + idx * 6 + face];
    float4 vClip = mul(float4(vWorldPos, 1.f), v.ViewProj);
    if (vClip.w <= 0.f)
        return 1.f;
    float3 vNDC = vClip.xyz / vClip.w;
    float2 vTileUV = vNDC.xy * float2(0.5f, -0.5f) + 0.5f;
    if (any(vTileUV < 0.f) || any(vTileUV > 1.f))
        return 1.f;

    float fReceiver = (fDist / fRng) - g_RenderTuning[0].fPointBias;
    return Shadow_PCF_Atlas(g_PointMap, g_PointSampler, vTileUV, v.AtlasRect, fReceiver,
                                (float) g_PointInfo.z, g_RenderTuning[0].fPointFilterTexels, vPixel);
}

#endif