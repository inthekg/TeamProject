#include "Particle_WBOIT.hlsli"

cbuffer TrailCB : register(b4)
{
    matrix g_matVP;
    float4 g_vFlags;
    float4 g_TEAP;
    float4 g_BaseTS;
    float4 g_PowDens;
    float4 g_NoiseTS;
    float4 g_Misc;
    float4 g_Misc2;
};

Texture2D       g_BaseTex       : register(t0);
Texture2D       g_NoiseTex      : register(t1);
Texture2D       g_SceneTex      : register(t3);

SamplerState    g_LinearSampler : register(s0);
SamplerState    g_ClampSampler  : register(s2);

struct VS_IN
{
    float3 vPosition : POSITION;
    float4 vColor : COLOR;
    float2 vTexcoord : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vColor : COLOR;
    float2 vTexcoord : TEXCOORD0;
    float4 projPos : TEXCOORD1;
};

VS_OUT VS_Main(VS_IN In)
{
    VS_OUT Out;
    
    Out.vPosition = mul(float4(In.vPosition, 1.f), g_matVP);
    Out.vColor = In.vColor;
    Out.vTexcoord = In.vTexcoord;
    Out.projPos = Out.vPosition;
    
    return Out;
}

float4 Shade_Trail(VS_OUT In)
{
    float time = g_TEAP.x;
    
    float2 baseUV = In.vTexcoord * g_BaseTS.xy + time * g_BaseTS.zw;
    float2 noiseUV = In.vTexcoord * g_NoiseTS.xy + time * g_NoiseTS.zw;
    
    float baseT = (g_vFlags.x > 0.5f) ? g_BaseTex.Sample(g_LinearSampler, baseUV).r : 1.f;
    float noiseT = (g_vFlags.y > 0.5f) ? g_NoiseTex.Sample(g_LinearSampler, noiseUV).r : 1.f;
    float baseP = pow(saturate(baseT), g_PowDens.x) * g_PowDens.y;
    float noiseP = pow(saturate(noiseT), g_PowDens.z) * g_PowDens.w;
    
    float pattern = baseP * noiseP;
    float3 emissive = In.vColor.rgb * pattern * g_TEAP.y;
    
    float3 outColor = emissive;
    if (g_TEAP.w > 0.5f)
    {
        float2 screenUV = In.vPosition.xy / g_Misc.zw;
        float2 offset;
        
        float2 gradY = float2(ddx(In.vTexcoord.y), ddy(In.vTexcoord.y));
        float fLen = max(length(gradY), 1e-6f);
        float2 acrossDir = gradY / fLen;
        float r = (In.vTexcoord.y - 0.5f) * 2.f;
        offset = -acrossDir * r * g_Misc.y / g_Misc.zw;
        
        float3 distorted = g_SceneTex.Sample(g_ClampSampler, screenUV + offset).rgb;
        outColor = distorted + emissive;
    }
    
    float ay = 1.f;
    if (g_vFlags.z > 0.5f)
    {
        float v = In.vTexcoord.y;
        int alphaYMode = (int) (g_vFlags.w + 0.5f);
        float pw = max(g_TEAP.z, 1e-5f);
        float gy;
        if (alphaYMode == 0)
            gy = 1.f - v;
        else if (alphaYMode == 1)
            gy = v;
        else
        {
            float m = clamp(g_Misc2.x, 0.02f, 0.98f);
            float d = (v < m) ? (m - v) / m : (v - m) / (1.f - m);
            gy = (alphaYMode == 2) ? (1.f - d) : d;
            if (v >= m)
                pw = max(g_Misc.x, 1e-5f);
        }
        ay = pow(saturate(gy), pw);
    }
    float opacity = In.vColor.a * ay;
    
    float4 c = float4(outColor, opacity);
   
    c.rgb *= c.a;
    
    return c;
}

float4 PS_Main(VS_OUT In) : SV_TARGET
{
    return Shade_Trail(In);
}

PS_OIT_OUT PS_Main_WBOIT(VS_OUT In)
{
    float4 c = Shade_Trail(In);

    float w = OIT_Weight(In.projPos.w);

    PS_OIT_OUT o;
    o.vAccum = float4(c.rgb, c.a) * w;
    o.vReveal = c.a;
    return o;
}