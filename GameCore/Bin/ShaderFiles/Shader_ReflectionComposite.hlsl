#include "Shader_FullScreen.hlsli"

Texture2D g_ReflectionTexture   : register(t29);
Texture2D g_DepthTexture        : register(t23);

float Linear_ViewZ(float2 uv)
{    
    return g_DepthTexture.Sample(g_PointSampler, uv).y * vCamPosition.w;
}

float3 Upsample_Reflection_Bilateral(float2 uv)
{
    float2 vHalfDim;
    g_ReflectionTexture.GetDimensions(vHalfDim.x, vHalfDim.y);
    float2 vTexel = 1.0f / vHalfDim;

    float fCenterZ = Linear_ViewZ(uv);
    float2 vOff[4] =
    {
        float2(-0.5f, -0.5f), float2(0.5f, -0.5f),
        float2(-0.5f, 0.5f), float2(0.5f, 0.5f)
    };

    float3 vSum = 0.f;
    float fWSum = 0.f;

    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float2 vTapUV = uv + vOff[i] * vTexel;
        float fDepthW = exp2(-abs(fCenterZ - Linear_ViewZ(vTapUV)) * 4.0f);
        float fW = fDepthW + 1e-4f;
        vSum += g_ReflectionTexture.SampleLevel(g_LinearClampSampler, vTapUV, 0).rgb * fW;
        fWSum += fW;
    }
    return vSum / fWSum;
}

VS_OUT VS_MAIN(uint iVertexID : SV_VertexID)
{
    VS_OUT Out;
    float2 vUV = float2((iVertexID << 1) & 2, iVertexID & 2);
    Out.vTexcoord = vUV;
    Out.vPosition = float4(vUV * float2(2.f, -2.f) + float2(-1.f, 1.f), 0.f, 1.f);
    return Out;
}

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;    
    Out.vColor = float4(Upsample_Reflection_Bilateral(In.vTexcoord), 0.f);
    return Out;
}


