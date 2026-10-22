#include "Shader_FullScreen.hlsli"

Texture2D g_GodRayTexture : register(t29);
Texture2D g_DepthTexture : register(t23);

cbuffer GodRayBlurBuffer : register(b9)
{
    float2 g_vBlurDir;
    float g_fDepthFalloff;
    float g_fBlurPad;
};


float Linear_ViewZ(float2 uv)
{
    return g_DepthTexture.Sample(g_PointSampler, uv).y * vCamPosition.w;
}


#define BLUR_COUNT 5

#if BLUR_COUNT > 5
static const int GR_TAPS = 13;
static const float GR_OFF[13] = { -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6 };
static const float GR_WEIGHT[13] =
{
    0.0561f, 0.1353f, 0.2780f, 0.4868f, 0.7261f, 0.9231f, 1.0000f,
    0.9231f, 0.7261f, 0.4868f, 0.2780f, 0.1353f, 0.0561f
};

float3 Blur_GodRay_Separable(float2 uv)
{
    float2 vHalfDim;
    g_GodRayTexture.GetDimensions(vHalfDim.x, vHalfDim.y);
    float2 vTexel = 1.0f / vHalfDim;

    float fCenterZ = Linear_ViewZ(uv);
    float3 vSum = 0.0f;
    float fWSum = 0.0f;

    [unroll]
    for (int i = 0; i < GR_TAPS; ++i)
    {
        float2 vTapUV = uv + g_vBlurDir * (GR_OFF[i] * vTexel);
        float fTapZ = Linear_ViewZ(vTapUV);
        float fDepthW = exp2(-abs(fCenterZ - fTapZ) * g_fDepthFalloff);
        float fW = GR_WEIGHT[i] * fDepthW + 1e-4f;
        vSum += g_GodRayTexture.SampleLevel(g_LinearClampSampler, vTapUV, 0).rgb * fW;
        fWSum += fW;
    }
    return vSum / fWSum;
}

#else 
static const int GR_TAPS = 5;
static const float GR_OFF[5] = { -2.0f, -1.0f, 0.0f, 1.0f, 2.0f };
static const float GR_WEIGHT[5] =
{
    0.0702702703f, 0.3162162162f, 0.2270270270f,0.3162162162f, 0.0702702703f
};


float3 Blur_GodRay_Separable(float2 uv)
{
    float2 vHalfDim;
    g_GodRayTexture.GetDimensions(vHalfDim.x, vHalfDim.y);
    float2 vTexel = 1.0f / vHalfDim;

    float fCenterZ = Linear_ViewZ(uv);
    float3 vSum = 0.0f;
    float fWSum = 0.0f;

    [unroll]
    for (int i = 0; i < GR_TAPS; ++i)
    {
        float2 vTapUV = uv + g_vBlurDir * (GR_OFF[i] * vTexel);
        float fTapZ = Linear_ViewZ(vTapUV);
        float fDepthW = exp2(-abs(fCenterZ - fTapZ) * g_fDepthFalloff);
        float fW = GR_WEIGHT[i] * fDepthW + 1e-4f;
        vSum += g_GodRayTexture.SampleLevel(g_LinearClampSampler, vTapUV, 0).rgb * fW;
        fWSum += fW;
    }
    return vSum / fWSum;
}

#endif

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out;
    Out.vColor = float4(Blur_GodRay_Separable(In.vTexcoord), 1.0f);
    return Out;
}