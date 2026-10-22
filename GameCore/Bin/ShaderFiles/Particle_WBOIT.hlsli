#ifndef PARTICLE_WBOIT_HLSLI
#define PARTICLE_WBOIT_HLSLI

float OIT_Weight(float z)
{
    return clamp(0.03f / (1e-5f + pow(abs(z) / 200.f, 4.f)), 1e-2f, 3e3f);
}

struct PS_OIT_OUT
{
    float4  vAccum : SV_Target0;
    float   vReveal : SV_Target1;
};

#endif