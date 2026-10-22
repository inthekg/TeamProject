#ifndef SHADER_IBL_HLSLI
#define SHADER_IBL_HLSLI

struct IBLData
{
    float fMaxMip;
    float fDiffuseIntensity;
    float fSpecularIntensity;
    float fPadding;
};

StructuredBuffer<IBLData> g_IBLData : register(t31);
#endif