#ifndef SHADER_POSTCOMMON_HLSLI
#define SHADER_POSTCOMMON_HLSLI
#include "Shader_Header.hlsli"


// ACES 톤맵 ( HDR ->LDR )
float3 ACES_Film(float3 x)
{
    x = max(x, 0.f);
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Henyey-GreenStein 위상함수 : 안개가 빛을 어느방향으로 더 튕기나.
float Henyey_GreeenStein(float cosT, float g)
{
    float g2 = g * g;
    float denom = 1.f + g2 - 2.f * g * cosT;
    return (1.f - g2) / (4.f * PI * pow(max(denom, 1e-4f), 1.5f));
}

float IGN(float2 px)
{
    return frac(52.9829189f * frac(dot(px, float2(0.06711056f, 0.00583715f))));
}
#endif