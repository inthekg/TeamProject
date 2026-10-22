#ifndef SHADER_RIMPROFILE_HLSLI
#define SHADER_RIMPROFILE_HLSLI

struct RimProfile
{
    float3 vColor;
    float fFill;
    float fIntensity;
    float3 _pad;
};

StructuredBuffer<RimProfile> g_RimProfiles : register(t40);

#endif