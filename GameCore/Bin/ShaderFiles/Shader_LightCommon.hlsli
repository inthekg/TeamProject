#ifndef SHADER_LIGHTCOMMON_HLSLI
#define SHADER_LIGHTCOMMON_HLSLI
#include "Shader_Header.hlsli"

float Distance_Attenuation(float fDist, float fRange)
{
    if (fDist >= fRange)
        return 0.f;
    float a = saturate(1.f - fDist / fRange);
    return a * a;
}

float Spot_ConeFactor(float3 vSurfaceToLight)
{
    float fCos = dot(normalize(vLight_Direction.xyz), -vSurfaceToLight);
    return smoothstep(fLight_OuterCos, fLight_InnerCos, fCos);
}

float3 Get_SurfaceToLight(float3 P, out float fDist)
{
    float3 vDir = vLight_Pos.xyz - P;
    fDist = length(vDir);
    return vDir / max(fDist, 1e-4f);
}
#endif