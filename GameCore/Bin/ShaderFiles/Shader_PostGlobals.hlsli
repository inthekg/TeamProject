#ifndef SHADER_POSTGLOBALS_HLSLI
#define SHADER_POSTGLOBALS_HLSLI

cbuffer PostGlobals : register(b12)
{
    float4 g_SMAA_RT_METRICS;

    float3 vFogColor;
    float fFogDensity;

    float fFogStart;
    float fFogHeight;
    float fFogHeightFalloff;
    int iFogVolumeCount;

    float fFeather;
    float fExposureScale;
    float fTime;
    float fReflectClipHeight;
}

#endif
