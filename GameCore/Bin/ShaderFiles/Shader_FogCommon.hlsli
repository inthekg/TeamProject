#ifndef SHADER_FOGCOMMON_HLSLI
#define SHADER_FOGCOMMON_HLSLI
#include "Shader_Header.hlsli"
#include "Shader_RenderTuning.hlsli"
#include "Shader_PostGlobals.hlsli"

#define FOG_MAX_DISTANCE 1000.f
#define FOG_HEIGHT_CLAMP 30.f

struct FogVolume
{
    float3 vCenter;
    float fRadius;

    float3 vColor;
    float fDensity;

    float3 vExtents;
    float fHeightFalloff;

    float3 vAxisX;
    int iShapeType;

    float3 vAxisY;
    float fHeightFalloffDirection;

    float3 vAxisZ;
    float fPad1;
};

StructuredBuffer<FogVolume> g_FogVolumes : register(t37);

float Integrate_HeightDensity(float fDensity, float fFalloff, float fYBase, float fHeightFalloffDirection, float fOriginY, float fDirY, float t0, float t1)
{
    float fSeg = t1 - t0;

    if (fSeg <= 0.f)
        return 0.f;

    if (fFalloff <= 1e-4f)
        return fDensity * fSeg;

    float fEnterY = clamp((fOriginY + fDirY * t0 - fYBase) * fHeightFalloffDirection, 0.f, FOG_HEIGHT_CLAMP);
    float fEnterDens = fDensity * exp(-fFalloff * fEnterY);

    if (abs(fDirY) > 1e-4f)
    {
        float fK = fFalloff * fDirY * fHeightFalloffDirection;
        return fEnterDens * (1.f - exp(-fK * fSeg)) / fK;
    }

    return fEnterDens * fSeg;
}

bool Intersect_FogSphere(FogVolume vol, float3 vOrigin, float3 vDir, float fMaxDist, float fFeatherAmount, out float t0, out float t1, out float fEdge)
{
    t0 = 0.f;
    t1 = 0.f;
    fEdge = 0.f;

    float3 oc = vOrigin - vol.vCenter;
    float fB = dot(oc, vDir);
    float fRadiusSq = vol.fRadius * vol.fRadius;
    float fC = dot(oc, oc) - fRadiusSq;
    float fDisc = fB * fB - fC;

    if (fDisc <= 0.f)
        return false;

    float fSq = sqrt(fDisc);
    t0 = max(-fB - fSq, 0.f);
    t1 = min(-fB + fSq, fMaxDist);

    if (t1 <= t0)
        return false;

    fEdge = smoothstep(0.f, max(fFeatherAmount, 1e-3f), fDisc / max(fRadiusSq, 1e-4f));

    return true;
}

bool Intersect_FogBox(FogVolume vol, float3 vOrigin, float3 vDir, float fMaxDist, float fFeatherAmount, out float t0, out float t1, out float fEdge)
{
    t0 = 0.f;
    t1 = 0.f;
    fEdge = 0.f;

    float3x3 matAxis = float3x3(vol.vAxisX, vol.vAxisY, vol.vAxisZ);

    float3 vLocalO = mul(matAxis, vOrigin - vol.vCenter);
    float3 vLocalD = mul(matAxis, vDir);

    float3 vSafeD = vLocalD;
    vSafeD.x = abs(vSafeD.x) < 1e-6f ? 1e-6f : vSafeD.x;
    vSafeD.y = abs(vSafeD.y) < 1e-6f ? 1e-6f : vSafeD.y;
    vSafeD.z = abs(vSafeD.z) < 1e-6f ? 1e-6f : vSafeD.z;

    float3 vExtent = max(vol.vExtents, 1e-3f);

    float3 vTa = (-vExtent - vLocalO) / vSafeD;
    float3 vTb = (vExtent - vLocalO) / vSafeD;

    float3 vTMin = min(vTa, vTb);
    float3 vTMax = max(vTa, vTb);

    t0 = max(max(vTMin.x, vTMin.y), max(vTMin.z, 0.f));
    t1 = min(min(vTMax.x, vTMax.y), min(vTMax.z, fMaxDist));

    if (t1 <= t0)
        return false;

    float3 vMid = vLocalO + vLocalD * ((t0 + t1) * 0.5f);
    float3 vFade = saturate((vExtent - abs(vMid)) / max(vExtent * max(fFeatherAmount, 1e-3f), 1e-4f));

    fEdge = min(min(vFade.x, vFade.y), vFade.z);

    return true;
}

float Compute_SceneFog(float3 vWorldPos, out float3 vFogColorOut)
{
    float3 vFogTint = g_RenderTuning[0].vFogColor;
    float fGlobalDensity = g_RenderTuning[0].fFogDensity;
    float fGlobalStart = g_RenderTuning[0].fFogStart;
    float fGlobalHeight = g_RenderTuning[0].fFogHeight;
    float fFeatherAmount = g_RenderTuning[0].fFogFeather;

    float3 vRay = vWorldPos - vCamPosition.xyz;
    float fDist = min(length(vRay), FOG_MAX_DISTANCE);
    float3 vDir = vRay / max(length(vRay), 1e-4f);

    float fA = min(fGlobalStart, fDist);
    float fB = fDist;
    float b = max(g_RenderTuning[0].fFogHeightFalloff, 1e-4f);

    float fCamAbove = clamp(vCamPosition.y - fGlobalHeight, -FOG_HEIGHT_CLAMP, FOG_HEIGHT_CLAMP);
    float fDensCam = fGlobalDensity * exp(-b * fCamAbove);

    float fTauGlobal;
    if (abs(vDir.y) > 1e-4f)
        fTauGlobal = fDensCam * (exp(-b * vDir.y * fA) - exp(-b * vDir.y * fB)) / (b * vDir.y);
    else
        fTauGlobal = fDensCam * (fB - fA);
    fTauGlobal = clamp(fTauGlobal, 0.f, 64.f);

    if (g_RenderTuning[0].iGlobalFogEnable == 0)
        fTauGlobal = 0.f;

    float fTauLocal = 0.f;
    float3 vLocalColorW = 0.f;
    [loop]
    for (int i = 0; i < iFogVolumeCount; ++i)
    {
        FogVolume vol = g_FogVolumes[i];

        float t0 = 0.f;
        float t1 = 0.f;
        float fEdge = 0.f;
        bool bHit = false;

        if (1 == vol.iShapeType)
            bHit = Intersect_FogBox(vol, vCamPosition.xyz, vDir, fDist, fFeatherAmount, t0, t1, fEdge);
        else
            bHit = Intersect_FogSphere(vol, vCamPosition.xyz, vDir, fDist, fFeatherAmount, t0, t1, fEdge);

        if (false == bHit)
            continue;

        float fHalfHeight = vol.fRadius;

        if (1 == vol.iShapeType)
            fHalfHeight = abs(vol.vAxisX.y) * vol.vExtents.x + abs(vol.vAxisY.y) * vol.vExtents.y + abs(vol.vAxisZ.y) * vol.vExtents.z; // 회전 Box의 실제 월드 Y 반높이를 사용한다.

        float fHeightFalloffDirection = vol.fHeightFalloffDirection < 0.f ? -1.f : 1.f;
        float fYBase = vol.vCenter.y - fHalfHeight * fHeightFalloffDirection;

        float fTauV = Integrate_HeightDensity(vol.fDensity, vol.fHeightFalloff, fYBase, fHeightFalloffDirection,
                                              vCamPosition.y, vDir.y, t0, t1) * fEdge;

        fTauV = clamp(fTauV, 0.f, 64.f);

        fTauLocal += fTauV;
        vLocalColorW += vol.vColor * fTauV;
    }

    float fTauTotal = fTauGlobal + fTauLocal;
    vFogColorOut = (fTauTotal > 1e-5f)
        ? (vFogTint * fTauGlobal + vLocalColorW) / fTauTotal
        : vFogTint;
    return saturate(1.f - exp(-fTauTotal));
}

float3 Apply_SceneFog(float3 vColor, float3 vWorldPos)
{
    float3 vFog;
    float fFog = Compute_SceneFog(vWorldPos, vFog);
    return lerp(vColor, vFog, fFog);
}
#endif
