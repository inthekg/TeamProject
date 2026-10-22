#ifndef SHADER_DISSOLVE_HLSLI
#define SHADER_DISSOLVE_HLSLI
#include "Shader_Header.hlsli"

static const float DISSOLVE_NOISE_SCALE = 6.f;
static const float DISSOLVE_EDGE_WIDTH = 0.12f;

float Dissolve_Hash(float3 p)
{
    p = frac(p * float3(0.1031f, 0.1030f, 0.0973f));
    p += dot(p, p.yzx + 33.33f);
    return frac((p.x + p.y) * p.z);
}

float Dissolve_Noise(float3 x)
{
    float3 i = floor(x);
    float3 f = frac(x);
    float3 u = f * f * (3.f - 2.f * f);
    
    float n000 = Dissolve_Hash(i + float3(0.f, 0.f, 0.f));
    float n100 = Dissolve_Hash(i + float3(1.f, 0.f, 0.f));
    float n010 = Dissolve_Hash(i + float3(0.f, 1.f, 0.f));
    float n110 = Dissolve_Hash(i + float3(1.f, 1.f, 0.f));
    float n001 = Dissolve_Hash(i + float3(0.f, 0.f, 1.f));
    float n101 = Dissolve_Hash(i + float3(1.f, 0.f, 1.f));
    float n011 = Dissolve_Hash(i + float3(0.f, 1.f, 1.f));
    float n111 = Dissolve_Hash(i + float3(1.f, 1.f, 1.f));

    float nx00 = lerp(n000, n100, u.x);
    float nx10 = lerp(n010, n110, u.x);
    float nx01 = lerp(n001, n101, u.x);
    float nx11 = lerp(n011, n111, u.x);
    float nxy0 = lerp(nx00, nx10, u.y);
    float nxy1 = lerp(nx01, nx11, u.y);
    return lerp(nxy0, nxy1, u.z);
}

float3 Compute_Dissolve(float3 vWorldPos)
{
    if (fDissolveAmount <= 0.0001f)
        return float3(0.f, 0.f, 0.f);
    
    float n = Dissolve_Noise(vWorldPos * DISSOLVE_NOISE_SCALE);
    float edge = DISSOLVE_EDGE_WIDTH;
    float cut = fDissolveAmount * (1.f + edge);
    
    if (n < cut)
        discard;
    
    float band = 1.f - saturate((n - cut) / edge);
    return band * vDissolveColor.rgb * vDissolveColor.a;
}

#endif