#include "Shader_FullScreen.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_PostCommon.hlsli"
#include "Shader_IBL.hlsli"
#include "Shader_RenderTuning.hlsli"

Texture2D   g_DiffuseTexture : register(t20);
Texture2D   g_NormalTexture  : register(t21);
Texture2D   g_ARMTexture     : register(t22);
Texture2D   g_DepthTexture   : register(t23);
Texture2D   g_LightTexture   : register(t24);
TextureCube g_PrefilterCube  : register(t26);
Texture2D   g_BRDFLUT        : register(t27);


static const int   SSR_MAX_STEPS    = 32;
static const int   SSR_BIN_STEPS    = 5;

float Sample_ViewZ(float2 uv)
{
    return g_DepthTexture.Sample(g_PointSampler, uv).y * vCamPosition.w;
}

float2 World_To_UV(float3 vWorld, out float fRayViewZ, out bool bInside)
{
    float4 vView = mul(float4(vWorld, 1.f), ViewMatrix);
    fRayViewZ = vView.z;
    float4 vClip = mul(vView, ProjMatrix);
    bInside = false;
    
    if(vClip.w <= 0.f)
        return float2(0, 0);
    float2 uv = vClip.xy / vClip.w;
    uv = uv * float2(0.5f, -0.5f) + 0.5f;
    bInside = all(uv >= 0.f) && all(uv <= 1.f);
    return uv;
}

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;

    float4 vDepthDesc = g_DepthTexture.Sample(g_PointSampler, In.vTexcoord);
    if (false == (vDepthDesc.x > 0.f && vDepthDesc.x < 1.f))
        discard;

    float3 vWorldPos = Reconstruct_WorldPos(In.vTexcoord, vDepthDesc);
    float3 vNormal = Decode_Normal(g_NormalTexture.Sample(g_PointSampler, In.vTexcoord));
    float4 vARM = g_ARMTexture.Sample(g_PointSampler, In.vTexcoord);
    float fAO = vARM.r;
    float fRoughness = vARM.g;
    float fMetalic = vARM.b;
    float3 vAlbedo = g_DiffuseTexture.Sample(g_PointSampler, In.vTexcoord).rgb;
    float3 vDirection = normalize(vCamPosition.xyz - vWorldPos);
    float NdotV = saturate(dot(vNormal, vDirection));
    float3 vReflect = reflect(-vDirection, vNormal);

    float fConf = 0.f;
    float2 vHitUV = 0.f;

    if (fRoughness < g_RenderTuning[0].fSSRRoughCutoff)
    {
        float3 vStart = vWorldPos + vNormal * g_RenderTuning[0].fSSRBias;
        float fStep = g_RenderTuning[0].fSSRMaxDistance / SSR_MAX_STEPS;
        float3 vPrev = vStart;
        bool bHit = false;

        [loop]
        for (int i = 1; i <= SSR_MAX_STEPS; ++i)
        {
            float3 vCur = vStart + vReflect * (fStep * i);
            float fRayZ;
            bool bIn;
            float2 uv = World_To_UV(vCur, fRayZ, bIn);
            if (false == bIn)
                break;

            float fDiff = fRayZ - Sample_ViewZ(uv);
            if (fDiff > 0.f && fDiff < g_RenderTuning[0].fSSRThickness + fStep)
            {
                float3 a = vPrev, b = vCur;
                [loop]
                for (int j = 0; j < SSR_BIN_STEPS; ++j)
                {
                    float3 m = (a + b) * 0.5f;
                    float mz;
                    bool mi;
                    float2 muv = World_To_UV(m, mz, mi);
                    if (false == mi)
                    {
                        b = m;
                        continue;
                    }

                    if (mz - Sample_ViewZ(muv) > 0.f)
                    {
                        b = m;
                        vHitUV = muv;
                    }
                    else
                    {
                        a = m;
                    }
                }
                bHit = true;
                break;
            }
            vPrev = vCur;
        }

        if (bHit)
        {
            float2 vEdge = smoothstep(0.0, 0.15, vHitUV) * smoothstep(0.0, 0.15, 1.0 - vHitUV);
            float fEdgeFade = vEdge.x * vEdge.y;
            float fRoughFade = saturate(1.f - fRoughness / g_RenderTuning[0].fSSRRoughCutoff);
            fConf = fEdgeFade * fRoughFade;
        }

    }

    float3 vSSR = g_LightTexture.SampleLevel(g_LinearClampSampler, vHitUV, 0).rgb;
    float3 vFallback = g_PrefilterCube.SampleLevel(g_LinearClampSampler, vReflect, fRoughness * g_IBLData[0].fMaxMip).rgb;
    float3 vRefl = lerp(vFallback, vSSR, fConf);

    float3 F0 = lerp(0.04f, vAlbedo, fMetalic);
    float2 brdf = g_BRDFLUT.Sample(g_LinearClampSampler, float2(NdotV, fRoughness)).rgb;
    float specOcc = saturate(pow(NdotV + fAO, exp2(-16.0 * fRoughness - 1.0)) - 1.0 + fAO);
    Out.vColor = float4(vRefl * (F0 * brdf.x + brdf.y) * specOcc * g_IBLData[0].fSpecularIntensity, 1.f);

    return Out;
}